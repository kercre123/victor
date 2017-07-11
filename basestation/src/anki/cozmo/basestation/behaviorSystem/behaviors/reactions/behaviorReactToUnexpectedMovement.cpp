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

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviors/reactions/behaviorReactToUnexpectedMovement.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"


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

  StartActing(new TriggerLiftSafeAnimationAction(robot, AnimationTrigger::ReactToUnexpectedMovement,
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
