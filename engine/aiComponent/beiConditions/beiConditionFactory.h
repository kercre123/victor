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

#include "engine/aiComponent/beiConditions/iBEICondition_fwd.h"

#include "util/helpers/noncopyable.h"

#include "json/json-forwards.h"

#include <map>

namespace Anki {
namespace Cozmo {
  
enum class BEIConditionType : uint8_t;

class BEIConditionFactory {
  friend class CustomBEIConditionHandleInternal;
public:
  static IBEIConditionPtr CreateBEICondition(const Json::Value& config, const std::string& ownerDebugLabel);

  // call this function to inject a custom code-defined condition into the factory. It can then be created in
  // the factory using "customCondition": "name" _rather than_ "conditionType". Note that "name" must be
  // unique among all custom conditions that are in scope simultaneously (so you shouldn't have to worry about
  // different behaviors having conditions of the same name). This function returns a handle that must be held
  // during the scope in which the custom condition will be created. This allows nesting conditions which can
  // mix data-defined and code-defined conditions. When the handle is destroyed, the entry will be removed
  static CustomBEIConditionHandle InjectCustomBEICondition(const std::string& conditionName, IBEIConditionPtr condition);
  
  // for conditions without config
  static IBEIConditionPtr CreateBEICondition(BEIConditionType type, const std::string& ownerDebugLabel);

  // Check if the given json looks like a condition (contains condition type or name keys)
  static bool IsValidCondition(const Json::Value& config);

private:

  static void RemoveCustomCondition(const std::string& name);

  static IBEIConditionPtr GetCustomCondition(const Json::Value& config, const std::string& ownerDebugLabel);

  // container for custom conditions. Will get cleaned up when the handles go out of scope
  static std::map< std::string, IBEIConditionPtr > _customConditionMap;
  
}; // class BEIConditionFactory

// handle class that can be used to manage injection of custom bei conditions. When it goes out of scope or is
// deleted, the condition will be removed
class CustomBEIConditionHandleInternal : private Util::noncopyable
{
  friend class BEIConditionFactory;  
public:
  ~CustomBEIConditionHandleInternal();

private:
  explicit CustomBEIConditionHandleInternal(const std::string& conditionName);

  std::string _conditionName;
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_BEIConditionFactory_H__
