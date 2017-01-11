/**
 * File: reactionTriggerStrategyMotorCalibration.cpp
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyMotorCalibration.h"
#include "anki/cozmo/basestation/behaviors/iBehavior.h"

namespace Anki {
namespace Cozmo {

namespace{
static const char* kTriggerStrategyName = "Trigger Strategy Motor Calibration";
}

ReactionTriggerStrategyMotorCalibration::ReactionTriggerStrategyMotorCalibration(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
  SubscribeToTags({
    EngineToGameTag::MotorCalibration
  });
}
  
  
bool ReactionTriggerStrategyMotorCalibration::ShouldTriggerBehavior(const Robot& robot, const IBehavior* behavior)
{
  if(_shouldComputationallySwitch){
    _shouldComputationallySwitch = false;
    return behavior->IsRunnable(ReactionTriggerConst::kNoPreReqs);
  }
  
  return false;
}


  
void ReactionTriggerStrategyMotorCalibration::AlwaysHandleInternal(const EngineToGameEvent& event, const Robot& robot)
{
  switch(event.GetData().GetTag())
  {
    case EngineToGameTag::MotorCalibration:
    {
      const MotorCalibration& msg = event.GetData().Get_MotorCalibration();
      if(msg.autoStarted && msg.calibStarted)
      {
        _shouldComputationallySwitch = true;
      }
      break;
    }
    default:
    {
      PRINT_NAMED_ERROR("BehaviorReactToMotorCalibration.ShouldRunForEvent.BadEventType",
                        "Calling ShouldRunForEvent with an event we don't care about, this is a bug");
      break;
    }
  }
}

  
} // namespace Cozmo
} // namespace Anki
