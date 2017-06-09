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
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/cozmoContext.h"
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
CONSOLE_VAR(f32, kDistanceFromMarker_mm, "Behavior.FeedingEat",  40.0f);
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
    return true;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorFeedingEat::InitInternal(Robot& robot)
{
  _shouldSendUpdateUI = false;
  TransitionToDrivingToFood(robot);
  return Result::RESULT_OK;
}
 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::StopInternal(Robot& robot)
{
  SendDelayedUIUpdateIfAppropriate(robot);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::TransitionToDrivingToFood(Robot& robot)
{
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  const bool isNeedSevere = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
  
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
  AnimationTrigger eatingAnim = UpdateNeedsStateAndCalculateAnimation(robot);
  
  uint32_t timeDrainCube_s = 0;
  RobotManager* robot_mgr = robot.GetContext()->GetRobotManager();
  if( robot_mgr->HasAnimationForTrigger(eatingAnim) )
  {
    // Extract the length of time that the animation will be playing for so that
    // it can be passed through to listeners
    const auto& animGroupName = robot_mgr->GetAnimationForTrigger(eatingAnim);
    const auto& animName = robot.GetAnimationStreamer().GetAnimationNameFromGroup(animGroupName, robot);
    Animation* eatingAnimRawPointer = robot.GetContext()->GetRobotManager()->GetCannedAnimations().GetAnimation(animName);
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
  
  StartActing(new TriggerAnimationAction(robot, eatingAnim));
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationTrigger BehaviorFeedingEat::UpdateNeedsStateAndCalculateAnimation(Robot& robot)
{
  NeedsState& currNeedState = robot.GetContext()->GetNeedsManager()->GetCurNeedsStateMutable();
  
  // Eating animation is dependent on both the current and post feeding energy level
  // We will grab the before state, update the needs manager as though cozmo has been
  // fed, and then grab the resolved state.  However, we don't send the DelayedUIUpdate
  // message until the actual feeding completes so that the user won't see the updated
  // energy state until feeding happens
  const bool isSeverePreFeeding = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
  const bool isWarningPreFeeding = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Warning);
  
  robot.GetContext()->GetNeedsManager()->RegisterNeedsActionCompleted(NeedsActionId::FeedBlue);
  
  const bool isSeverePostFeeding = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Critical);
  const bool isWarningPostFeeding = currNeedState.IsNeedAtBracket(NeedId::Energy, NeedBracketId::Warning);
  
  AnimationTrigger bestAnimation;
  if(isSeverePreFeeding && isSeverePostFeeding){
    bestAnimation = AnimationTrigger::FeedingAteNotFullEnough_Severe;
  }else if(isSeverePreFeeding && isWarningPostFeeding){
    bestAnimation = AnimationTrigger::FeedingAteFullEnough_Severe;
  }else if(isWarningPreFeeding && isWarningPostFeeding){
    bestAnimation = AnimationTrigger::FeedingAteNotFullEnough_Normal;
  }else{
    bestAnimation = AnimationTrigger::FeedingAteFullEnough_Normal;
  }
  
  _shouldSendUpdateUI = true;
  return bestAnimation;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorFeedingEat::SendDelayedUIUpdateIfAppropriate(Robot& robot)
{
  if(robot.HasExternalInterface() && _shouldSendUpdateUI){
    ExternalInterface::DelayedUIUpdateForAction delayedUpdate;
    delayedUpdate.actionCausingTheUpdate = NeedsActionId::FeedBlue;
    robot.GetExternalInterface()->BroadcastToGame<
    ExternalInterface::DelayedUIUpdateForAction>(delayedUpdate);
  }
}

  
} // namespace Cozmo
} // namespace Anki

