/**
 * File: reactionTriggerStrategyPyramid.cpp
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Reaction Trigger strategy for responding to
 *
 * Copyright: Anki, Inc. 2016
 *
 *
 **/

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyPyramidInitialDetection.h"

namespace Anki {
namespace Cozmo {
  
namespace{
static const char* kTriggerStrategyName = "Trigger Strategy Pyramid initial detection";
}

ReactionTriggerStrategyPyramidInitialDetection::ReactionTriggerStrategyPyramidInitialDetection(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
  
  
}

  
} // namespace Cozmo
} // namespace Anki
