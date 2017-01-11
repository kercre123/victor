/**
 * File: reactionTriggerStrategySparked.cpp
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

#include "anki/cozmo/basestation/behaviorSystem/reactionTriggerStrategies/reactionTriggerStrategyStackOfCubesInitialDetection.h"

namespace Anki {
namespace Cozmo {

namespace{
static const char* kTriggerStrategyName = "Trigger strategy stack of cubes";
}

  
ReactionTriggerStrategyStackOfCubesInitialDetection::ReactionTriggerStrategyStackOfCubesInitialDetection(Robot& robot, const Json::Value& config)
: IReactionTriggerStrategy(robot, config, kTriggerStrategyName)
{
  
  
}


} // namespace Cozmo
} // namespace Anki
