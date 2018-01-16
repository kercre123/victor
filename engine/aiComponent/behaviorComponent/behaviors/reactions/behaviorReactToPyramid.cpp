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

#include "engine/aiComponent/behaviorComponent/behaviors/reactions/behaviorReactToPyramid.h"

#include "engine/blockWorld/blockConfigurationManager.h"
#include "engine/blockWorld/blockWorld.h"

#include "coretech/common/engine/utils/timer.h"

namespace Anki {
namespace Cozmo {
  
namespace{
const constexpr float kTimeBetweenReactions_s = 100.f;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
BehaviorReactToPyramid::BehaviorReactToPyramid(const Json::Value& config)
: ICozmoBehavior(config)
, _nextValidReactionTime_s(0)
{
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool BehaviorReactToPyramid::WantsToBeActivatedBehavior() const
{
  using namespace BlockConfigurations;
  auto allPyramids = GetBEI().GetBlockWorld().GetBlockConfigurationManager().GetPyramidCache().GetPyramids();
  if(allPyramids.size() > 0){
    const auto currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
    if(currentTime > _nextValidReactionTime_s){
      return true;
    }
  }
  
  return false;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void BehaviorReactToPyramid::OnBehaviorActivated()
{
  _nextValidReactionTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + kTimeBetweenReactions_s;
  
}

} // namespace Cozmo
} // namespace Anki
