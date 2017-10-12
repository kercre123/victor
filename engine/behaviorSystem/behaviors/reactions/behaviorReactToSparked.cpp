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

#include "engine/behaviorSystem/behaviors/reactions/behaviorReactToSparked.h"

#include "anki/common/basestation/utils/timer.h"
#include "engine/behaviorSystem/behaviorManager.h"
#include "engine/moodSystem/moodManager.h"
#include "engine/robot.h"


namespace Anki {
namespace Cozmo {
  
BehaviorReactToSparked::BehaviorReactToSparked(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
{
  
  
}

Result BehaviorReactToSparked::InitInternal(Robot& robot)
{
  const float currTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  robot.GetMoodManager().TriggerEmotionEvent("SparkPending", currTime_s);
  return Result::RESULT_OK;
}

bool BehaviorReactToSparked::IsRunnableInternal(const Robot& robot) const
{
  return true;
}

} // namespace Cozmo
} // namespace Anki

