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

#include "anki/common/basestation/jsonTools.h"

namespace Anki {
namespace Cozmo {
// forward declarations
class IReactionTriggerStrategy;
class Robot;
enum class ReactionTrigger : uint8_t;
  
class ReactionTriggerStrategyFactory{
public:
  static IReactionTriggerStrategy* CreateReactionTriggerStrategy(Robot& robot,
                                                          const Json::Value& config,
                                                          ReactionTrigger trigger);
  
}; // class ReactionTriggerStrategyFactory
} // namespace Cozmo
} // namespace Anki

#endif //
