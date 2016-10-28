/**
 * File: behaviorReactToStackOfCubes.cpp
 *
 * Author: Kevin M. Karol
 * Created: 2016-10-27
 *
 * Description: Cozmo reacts to seeing a stack of cubes
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviors/reactionary/behaviorReactToStackOfCubes.h"

#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/blockWorld/blockConfigurationManager.h"
#include "anki/cozmo/basestation/blockWorld/blockWorld.h"

#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const constexpr float kTimeBetweenReactions_s = 100.f;
}
  
BehaviorReactToStackOfCubes::BehaviorReactToStackOfCubes(Robot& robot, const Json::Value& config)
: IReactionaryBehavior(robot, config)
{
  SetDefaultName("ReactToStackOfCubes");
  
}
  
bool BehaviorReactToStackOfCubes::IsRunnableInternalReactionary(const Robot& robot) const
{
  using namespace BlockConfigurations;
  auto allPyramids = robot.GetBlockWorld().GetBlockConfigurationManager().GetConfigurationsForType(ConfigurationType::Pyramid);
  if(allPyramids.size() > 0){
    TimeStamp_t currentTime = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    if(currentTime > _nextValidReactionTime_s){
      return true;
    }
  }
  
  return false;
}

Result BehaviorReactToStackOfCubes::InitInternalReactionary(Robot& robot)
{
  _nextValidReactionTime_s = BaseStationTimer::getInstance()->GetCurrentTimeStamp() + kTimeBetweenReactions_s;
  return Result::RESULT_OK;
}

} // namespace Cozmo
} // namespace Anki
