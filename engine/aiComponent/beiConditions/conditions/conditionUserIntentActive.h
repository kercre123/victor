/**
* File: conditionUserIntentActive.h
*
* Author: ross
* Created: 2018 May 16
*
* Description: Condition to check if there is an active intent that matches one or more user intents
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionUserIntentActive_H__
#define __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionUserIntentActive_H__

#include "engine/aiComponent/beiConditions/conditions/iConditionUserIntent.h"

namespace Anki {
namespace Cozmo {

class ConditionUserIntentActive : public IConditionUserIntent
{
public:
  
  // ctor for use with json and factory. provide a list of ways to compare to an intent. if any
  // are true, this condition will be true. See method for details
  explicit ConditionUserIntentActive( const Json::Value& config );
  
  // special constructor for which you MUST immediately use an Add* method from the base class
  ConditionUserIntentActive();
  
  // helper to build the sort of json you'd find in a file
  static Json::Value GenerateConfig( const Json::Value& config );
};

} // namespace Cozmo
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_WantsToRunStrategies_ConditionUserIntentActive_H__
