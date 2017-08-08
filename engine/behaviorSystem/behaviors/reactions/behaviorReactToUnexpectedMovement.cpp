/**
 * File: behaviorReactToUnexpectedMovement.cpp
 *
 * Author: Al Chaussee
 * Created: 7/11/2016
 *
 * Description: Behavior for reacting to unexpected movement like being spun while moving
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "engine/behaviorSystem/behaviors/reactions/behaviorReactToUnexpectedMovement.h"

#include "engine/actions/animActions.h"
#include "engine/aiComponent/aiComponent.h"
#include "engine/aiComponent/AIWhiteboard.h"
#include "engine/robot.h"
#include "engine/robotManager.h"
#include "engine/moodSystem/moodManager.h"


namespace Anki {
namespace Cozmo {
  
BehaviorReactToUnexpectedMovement::BehaviorReactToUnexpectedMovement(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{  
  SubscribeToTags({
    EngineToGameTag::UnexpectedMovement
  });
}

bool BehaviorReactToUnexpectedMovement::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return true;
}

Result BehaviorReactToUnexpectedMovement::InitInternal(Robot& robot)
{
  // Make Cozmo more frustrated if he keeps running into things/being turned
  robot.GetMoodManager().TriggerEmotionEvent("ReactToUnexpectedMovement", MoodManager::GetCurrentTimeInSeconds());
  
  // Lock the wheels if the unexpected movement is behind us so we don't drive backward and delete the created obstacle
  // TODO: Consider using a different animation that drives forward instead of backward? (COZMO-13035)
  const u8 tracksToLock = Util::EnumToUnderlying(_unexpectedMovementSide == UnexpectedMovementSide::BACK ?
                                                 AnimTrackFlag::BODY_TRACK :
                                                 AnimTrackFlag::NO_TRACKS);
  
  const u32  kNumLoops = 1;
  const bool kInterruptRunning = true;
  
  AnimationTrigger reactionAnimation = AnimationTrigger::ReactToUnexpectedMovement;
  
  NeedId expressedNeed = robot.GetAIComponent().GetWhiteboard().GetSevereNeedExpression();
  if(expressedNeed == NeedId::Energy){
    reactionAnimation = AnimationTrigger::ReactToUnexpectedMovement_Severe_Energy;
  }else if(expressedNeed == NeedId::Repair){
    reactionAnimation = AnimationTrigger::ReactToUnexpectedMovement_Severe_Repair;
  }
  
  StartActing(new TriggerLiftSafeAnimationAction(robot, reactionAnimation,
                                                 kNumLoops, kInterruptRunning, tracksToLock), [this]()
  {
    BehaviorObjectiveAchieved(BehaviorObjective::ReactedToUnexpectedMovement);
  });
  
  return RESULT_OK;
}
  
void BehaviorReactToUnexpectedMovement::AlwaysHandle(const EngineToGameEvent& event, const Robot& robot)
{
  _unexpectedMovementSide = event.GetData().Get_UnexpectedMovement().movementSide;
}
  
}
}
