/**
 * File: AIGoalStrategyObjectTapInteraction
 *
 * Author: Al Chaussee
 * Created: 10/28/2016
 *
 * Description: Specific strategy for doing things with a double tapped object.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#include "AIGoalStrategyObjectTapInteraction.h"

#include "anki/common/basestation/utils/timer.h"
#include "anki/cozmo/basestation/ankiEventUtil.h"

namespace Anki {
namespace Cozmo {

  AIGoalStrategyObjectTapInteraction::AIGoalStrategyObjectTapInteraction(Robot& robot, const Json::Value& config)
  : Anki::Cozmo::IAIGoalStrategy(config)
  {
  
  }
  
} // namespace
} // namespace
