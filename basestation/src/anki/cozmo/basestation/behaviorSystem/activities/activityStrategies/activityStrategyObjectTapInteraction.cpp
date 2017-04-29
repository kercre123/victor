/**
 * File: ActivityStrategyObjectTapInteraction.cpp
 *
 * Author: Al Chaussee
 * Created: 10/28/2016
 *
 * Description: Specific strategy for doing things with a double tapped object.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/activities/activityStrategies/activityStrategyObjectTapInteraction.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"

namespace Anki {
namespace Cozmo {

ActivityStrategyObjectTapInteraction::ActivityStrategyObjectTapInteraction(Robot& robot, const Json::Value& config)
: Anki::Cozmo::IActivityStrategy(config)
{

}
  
} // namespace
} // namespace
