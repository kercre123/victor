/**
 * File: behaviorReactToPyramid.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2016-10-27
 *
 * Description: Cozmo reacts to seeing a pyramid
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToPyramid.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/behaviorSystem/behaviorPreReqs/behaviorPreReqRobot.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"

#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const constexpr float kTimeBetweenReactions_s = 100.f;
}
  
BehaviorReactToPyramid::BehaviorReactToPyramid(Robot& robot, const Json::Value& config)
: IBehavior(robot, config)
, _nextValidReactionTime_s(0)
{
  SetDefaultName("ReactToPyramid");
}
  
bool BehaviorReactToPyramid::IsRunnableInternal(const BehaviorPreReqRobot& preReqData) const
{
  using namespace BlockConfigurations;
  auto allPyramids = preReqData.GetRobot().GetBlockWorld().GetBlockConfigurationManager().GetPyramidCache().GetPyramids();
  if(allPyramids.size() > 0){
    const auto currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if(currentTime > _nextValidReactionTime_s){
      return true;
    }
  }
  
  return false;
}

Result BehaviorReactToPyramid::InitInternal(Robot& robot)
{
  _nextValidReactionTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + kTimeBetweenReactions_s;
  return Result::RESULT_OK;
}

} // namespace Cozmo
} // namespace Anki
