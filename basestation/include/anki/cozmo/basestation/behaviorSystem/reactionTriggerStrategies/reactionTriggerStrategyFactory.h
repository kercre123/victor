/**
 * File: reactionTriggerStrategyFactory.h
 *
 * Author: Kevin M. Karol
 * Created: 12/08/16
 *
 * Description: Factory for creating reactionTriggerStrategy
 *
 * Copyright: Anki, Inc. 2016
 *
 **/

#ifndef __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyFactory_H__
#define __Cozmo_Basestation_BehaviorSystem_ReactionTriggerStrategyFactory_H__

#include "clad/types/behaviorTypes.h"
#include "anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {
class IReactionTriggerStrategy;
class Robot;
  
class ReactionTriggerStrategyFactory{
public:
  static IReactionTriggerStrategy* CreateReactionTriggerStrategy(Robot& robot,
                                                          const Json::Value& config,
                                                          ReactionTrigger trigger);
  
}; // class ReactionTriggerStrategyFactory
} // namespace Cozmo
} // namespace Anki

#endif //
