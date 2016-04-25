/**
 * File: behaviorReactToStop.h
 *
 * Author: Brad Neuman
 * Created: 2016-04-18
 *
 * Description: Reactionary behavior for the Stop event, which may be shortly followed by a cliff or pickup event
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/behaviorReactToStop.h"

#include "anki/cozmo/basestation/actions/basicActions.h"
#include "anki/cozmo/basestation/behaviorSystem/AIWhiteboard.h"
#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/shared/cozmoConfig.h"
#include "clad/externalInterface/messageEngineToGame.h"



namespace Anki {
namespace Cozmo {
  
using namespace ExternalInterface;
  
BehaviorReactToStop::BehaviorReactToStop(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToStop");

  // These are the tags that should trigger this behavior to be switched to immediately
  SubscribeToTriggerTags({
    EngineToGameTag::RobotStopped
  });
}

bool BehaviorReactToStop::IsRunnable(const Robot& robot) const
{
  // if we aren't allowed to react to cliffs, no reason to run this behavior since it is the job of the
  // behavior handling the cliff message to do it
  return robot.GetBehaviorManager().GetWhiteboard().IsCliffReactionEnabled();
}

Result BehaviorReactToStop::InitInternal(Robot& robot)
{
  // TODO:(bn) use an animation instead?

  // in case latency spiked between the Stop and Cliff message, add a small extra delay
  const float latencyDelay_s = 0.05f;
  const float waitTime_s = (1.0 / 1000.0 ) * CLIFF_EVENT_DELAY_MS + latencyDelay_s;
    
  StartActing(new WaitAction(robot, waitTime_s));
  PRINT_NAMED_INFO("BehaviorReactToStop.Init",
                   "Pausing to give time for a cliff or pickup event");
  
  return Result::RESULT_OK;
}
  
void BehaviorReactToStop::StopInternal(Robot& robot)
{
}

} // namespace Cozmo
} // namespace Anki
