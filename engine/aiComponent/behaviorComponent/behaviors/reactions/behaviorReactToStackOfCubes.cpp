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

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToStackOfCubes.h"

#include "engine/robot.h"
#include "engine/blockWorld/blockConfigurationManager.h"
#include "engine/blockWorld/blockWorld.h"

#include "anki/common/basestation/utils/timer.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const constexpr float kTimeBetweenReactions_s = 100.f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToStackOfCubes::BehaviorReactToStackOfCubes(const Json::Value& config)
: IBehavior(config)
{  
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToStackOfCubes::WantsToBeActivatedBehavior(BehaviorExternalInterface& behaviorExternalInterface) const
{
  using namespace BlockConfigurations;
  auto allPyramids = behaviorExternalInterface.GetBlockWorld().GetBlockConfigurationManager().GetStackCache().GetStacks();
  if(allPyramids.size() > 0){
    auto currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if(currentTime > _nextValidReactionTime_s){
      return true;
    }
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToStackOfCubes::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _nextValidReactionTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + kTimeBetweenReactions_s;
  return Result::RESULT_OK;
}

} // namespace Cozmo
} // namespace Anki
