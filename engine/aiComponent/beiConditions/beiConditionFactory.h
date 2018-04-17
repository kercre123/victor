/**
* File: stateConceptStrategyFactory.h
*
* Author: Kevin M. Karol
* Created: 6/03/17
*
* Description: Factory for creating wantsToRunStrategy
*
* Copyright: Anki, Inc. 2017
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_BEIConditionFactory_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_BEIConditionFactory_H__

#include "engine/aiComponent/aiComponents_fwd.h"
#include "engine/aiComponent/beiConditions/iBEICondition_fwd.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "util/helpers/noncopyable.h"

#include "json/json-forwards.h"

namespace Anki {
namespace Cozmo {

class CustomBEIConditionContainer;
enum class BEIConditionType : uint8_t;


class BEIConditionFactory : public IDependencyManagedComponent<AIComponentID>,  
                            private Util::noncopyable
{
public:
  BEIConditionFactory();
  ~BEIConditionFactory();

  IBEIConditionPtr CreateBEICondition(const Json::Value& config, const std::string& ownerDebugLabel);

  // for conditions without config
  IBEIConditionPtr CreateBEICondition(BEIConditionType type, const std::string& ownerDebugLabel);
  
}; // class BEIConditionFactory
  
} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_BEIConditionFactory_H__
