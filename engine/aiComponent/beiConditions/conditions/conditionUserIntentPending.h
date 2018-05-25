/**
* File: conditionUserIntentPending.h
*
* Author: ross
* Created: 2018 Feb 13
*
* Description: Condition to check if one or more user intent(s) are pending
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionUserIntentPending_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionUserIntentPending_H__

#include "engine/aiComponent/beiConditions/conditions/iConditionUserIntent.h"

namespace Anki {
namespace Cozmo {

class ConditionUserIntentPending : public IConditionUserIntent
{
public:
  
  // ctor for use with json and factory. provide a list of ways to compare to an intent. if any
  // are true, this condition will be true. See method for details
  explicit ConditionUserIntentPending( const Json::Value& config );
  
  // special constructor for which you MUST immediately use an Add* method from the base class
  explicit ConditionUserIntentPending( const std::string& ownerDebugLabel );
  
  // helper to build the sort of json you'd find in a file
  static Json::Value GenerateConfig( const Json::Value& config );
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionUserIntentPending_H__
