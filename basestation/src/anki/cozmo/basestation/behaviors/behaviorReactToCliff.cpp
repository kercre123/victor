/**
 * File: behaviorReactToCliff.cpp
 *
 * Author: Kevin
 * Created: 10/16/15
 *
 * Description: Behavior for immediately responding to a detected cliff
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "anki/cozmo/basestation/actions/animActions.h"
#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorWhiteboard.h"
#include "anki/cozmo/basestation/behaviors/behaviorReactToCliff.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/moodSystem/moodManager.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
static const char* kCliffReactAnimName = "anim_VS_loco_cliffReact";

BehaviorReactToCliff::BehaviorReactToCliff(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToCliff");

  // These are the tags that should trigger this behavior to be switched to immediately
  SubscribeToTriggerTags({
    EngineToGameTag::CliffEvent
  });
}

bool BehaviorReactToCliff::IsRunnable(const Robot& robot) const
{
  return robot.GetBehaviorManager().GetWhiteboard().IsCliffReactionEnabled();
}

Result BehaviorReactToCliff::InitInternal(Robot& robot)
{
  robot.GetMoodManager().TriggerEmotionEvent("CliffReact", MoodManager::GetCurrentTimeInSeconds());

  StartActing(new PlayAnimationAction(robot, kCliffReactAnimName));
  
  return Result::RESULT_OK;
}
  
void BehaviorReactToCliff::StopInternal(Robot& robot)
{
}

bool BehaviorReactToCliff::ShouldRunForEvent(const ExternalInterface::MessageEngineToGame& event) const
{
  if( event.GetTag() != MessageEngineToGameTag::CliffEvent ) {
    PRINT_NAMED_ERROR("BehaviorReactToCliff.ShouldRunForEvent.BadEventType",
                      "Calling ShouldRunForEvent with an event we don't care about, this is a bug");
    return false;
  }
  
  return event.Get_CliffEvent().detected;
} 

} // namespace Cozmo
} // namespace Anki
