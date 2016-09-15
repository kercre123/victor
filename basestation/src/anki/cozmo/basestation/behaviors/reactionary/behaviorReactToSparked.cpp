/**
 * File: behaviorReactToSparked.cpp
 *
 * Author:  Kevin M. Karol
 * Created: 2016-09-13
 *
 * Description:  Reaction that cancels other reactions when cozmo is sparked
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToSparked.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/behaviorManager.h"


namespace Anki {
namespace Cozmo {
  
BehaviorReactToSparked::BehaviorReactToSparked(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToSparked");
  
  
}
  
bool BehaviorReactToSparked::ShouldComputationallySwitch(const Robot& robot)
{
  const IBehavior* currentBehavior = robot.GetBehaviorManager().GetCurrentBehavior();
  if(currentBehavior == nullptr){
    return false;
  }
  
  const bool cancelCurrentReaction = currentBehavior->IsReactionary()
                                         && robot.GetBehaviorManager().ShouldSwitchToSpark();
  
  const bool behaviorWhitelisted = currentBehavior->GetType() == BehaviorType::ReactToCliff;
  
  return cancelCurrentReaction && !behaviorWhitelisted;
}

Result BehaviorReactToSparked::InitInternalReactionary(Robot& robot)
{
  return Result::RESULT_OK;
}

bool BehaviorReactToSparked::IsRunnableInternalReactionary(const Robot& robot) const
{
  return true;
}

} // namespace Cozmo
} // namespace Anki

