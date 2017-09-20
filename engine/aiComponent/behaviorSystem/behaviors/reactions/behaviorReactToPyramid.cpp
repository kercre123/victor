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

#include "engine/aiComponent/behaviorSystem/behaviors/reactions/behaviorReactToPyramid.h"

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
BehaviorReactToPyramid::BehaviorReactToPyramid(const Json::Value& config)
: IBehavior(config)
, _nextValidReactionTime_s(0)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToPyramid::IsRunnableInternal(BehaviorExternalInterface& behaviorExternalInterface) const
{
  using namespace BlockConfigurations;
  auto allPyramids = behaviorExternalInterface.GetBlockWorld().GetBlockConfigurationManager().GetPyramidCache().GetPyramids();
  if(allPyramids.size() > 0){
    const auto currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if(currentTime > _nextValidReactionTime_s){
      return true;
    }
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result BehaviorReactToPyramid::OnBehaviorActivated(BehaviorExternalInterface& behaviorExternalInterface)
{
  _nextValidReactionTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + kTimeBetweenReactions_s;
  return Result::RESULT_OK;
}

} // namespace Cozmo
} // namespace Anki
