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
: IBehavior(robot, config)
{
  SetDefaultName("ReactToSparked");
  
  
}

Result BehaviorReactToSparked::InitInternal(Robot& robot)
{
  return Result::RESULT_OK;
}

bool BehaviorReactToSparked::IsRunnableInternal(const BehaviorPreReqNone& preReqData) const
{
  return true;
}

} // namespace Cozmo
} // namespace Anki

